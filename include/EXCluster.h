#ifndef EXCLUSTER_H
#define EXCLUSTER_H
#include "types.h"
#include "EXVector.h"
#include "EXMatrix.h"

class EXGDiData;
class EXGeoHeader;

// Two-seed random stream (sizeof 0xc): each call steps the next seed of the pair through a
// shift-register and returns its mantissa as a float in [0,1).
class VU_Randf {
public:
    void SetSeed(int seed)
    {
        u32 t[2];
        t[0] = seed * 0x19660d + 0x3c6ef35f;
        t[1] = t[0] * 0x19660d + 0x3c6ef35f;
        m_seed[0] = t[0];
        m_seed[1] = t[1];
        m_index = 0;
    }
    float Randf()
    {
        if (++m_index >= 2) m_index = 0;
        u32 *p = &m_seed[m_index];
        u32 v = *p;
        *p = (((v >> 18) ^ v) >> 4 & 1) | (v + v);
        u32 bits = (m_seed[m_index] & 0x7fffff) | 0x3f800000;
        return *(float *)&bits - 1.0f;
    }
    u32 m_seed[2];  // 0x00
    int m_index;    // 0x08
};

// Value as a function of the normalised distance from the cluster centre: a constant line
// plus a random line scaled by a [0,1) random number (sizeof 0x10).
class ExClusterDistanceFn {
public:
    void SetCoefficents(float *c, float *r)
    {
        m_const[0] = c[0];
        m_const[1] = c[1];
        m_rand[0] = r[0];
        m_rand[1] = r[1];
    }
    float EvaluateConst(float d) const { return d * m_const[1] + m_const[0]; }
    float EvaluateRand(float d) const { return d * m_rand[1] + m_rand[0]; }
    float Evaluate(float d, float r) const { return (d * m_rand[1] + m_rand[0]) * r + d * m_const[1] + m_const[0]; }
    float m_const[2];
    float m_rand[2];
};

// Map cluster record: a scatter of one entity over a quadratic ground patch.
// m_flags: 1 circular (else square) area, 4 no ground alignment, 0x100 scale, 0x200 alpha,
// 0x400/0x800/0x1000 red/green/blue distance functions.
struct EXGeoCluster {
    EXVector3 m_pos;                // 0x00
    EXVector3 m_rot;                // 0x0c zxy angles
    u8 m_18[0x24 - 0x18];
    u16 m_flags;                    // 0x24
    u16 m_count;                    // 0x26
    int m_seed;                     // 0x28
    u32 m_entity;                   // 0x2c entity hash
    float m_sizeX;                  // 0x30
    float m_sizeZ;                  // 0x34
    float m_yawRange;               // 0x38 degrees
    float m_tiltRange;              // 0x3c degrees
    float m_ground[6];              // 0x40 y = c0 + c1 x + c2 x^2 + c3 z + c4 z^2 + c5 xz
    ExClusterDistanceFn m_alpha;    // 0x58
    ExClusterDistanceFn m_scale;    // 0x68
    ExClusterDistanceFn m_red;      // 0x78
    ExClusterDistanceFn m_green;    // 0x88
    ExClusterDistanceFn m_blue;     // 0x98
};

// Runtime instance list of one cluster (sizeof 0x1c).
class EXCluster {
public:
    EXCluster();
    ~EXCluster();
    void Draw(EXGeoHeader *pHdr, EXGeoCluster *pGeo);
protected:
    void GenerateInstance(EXGeoCluster *pGeo);
    void GenerateInstanceAlignment(EXGeoCluster *pGeo);
    void GenerateFinalInstanceMatrix(EXGeoCluster *pGeo);
    void BuildPreCalculatedPositions(EXGeoCluster *pGeo);
public:
    VU_Randf m_rand;        // 0x00
    EXGDiData *m_pDrawData; // 0x0c one per instance
    EXMatrix *m_pMatrix;    // 0x10
    float *m_pDistance;     // 0x14 squared normalised distance (build only)
    EXVector *m_pColour;    // 0x18 rgba multiplier
};

class EXClusterArray {
public:
    EXClusterArray() { m_pData = 0; m_count = 0; }
    ~EXClusterArray();
    void Init(unsigned int n);
    void RemoveAll();
    EXCluster *Clusters() const { return m_pData; }
    EXCluster *m_pData;
    u32 m_count;
};

#endif
