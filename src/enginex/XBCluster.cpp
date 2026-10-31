#include "EXCluster.h"
#include "EXMemory.h"
#include "EXBaseDisplay.h"
#include "EXBaseRenderEnv.h"
#include "EXGeoHeader.h"
#include "EXGeoEntity.h"
#include "XBGeoDrawInfo.h"
#include <math.h>

#define CLUSTER_DEG2RAD 0.017453292f

static __forceinline EXMaths::vec_xyzw ClusterAddXYZ(const EXMaths::vec_xyzw &a, const EXVector3 &b)
{
    EXMaths::vec_xyzw r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    r.w = a.w;
    return r;
}

EXCluster::EXCluster()
{
    m_pDrawData = 0;
    m_pMatrix = 0;
    m_pDistance = 0;
    m_pColour = 0;
}

EXCluster::~EXCluster()
{
    if (m_pDrawData) delete[] m_pDrawData;
    m_pDrawData = 0;
    if (m_pMatrix) EXFree(m_pMatrix);
    m_pMatrix = 0;
    if (m_pColour) EXFree(m_pColour);
    m_pColour = 0;
}

void EXClusterArray::Init(unsigned int n)
{
    m_pData = new EXCluster[n];
    m_count = n;
}

void EXClusterArray::RemoveAll()
{
    if (m_pData) {
        delete[] m_pData;
        m_pData = 0;
        m_count = 0;
    }
}

void EXCluster::GenerateInstanceAlignment(EXGeoCluster *pGeo)
{
    if (pGeo->m_flags & 4) return;
    for (int i = 0; i < pGeo->m_count; i++) {
        EXMatrix rx, rz;
        float a1 = (float)atan(pGeo->m_ground[2] * m_pMatrix[i].r[3].x * 2.0f + m_pMatrix[i].r[3].z * pGeo->m_ground[5] + pGeo->m_ground[1]);
        float a2 = -(float)atan(pGeo->m_ground[5] * m_pMatrix[i].r[3].x + m_pMatrix[i].r[3].z * pGeo->m_ground[4] * 2.0f + pGeo->m_ground[3]);
        rz.SetFromRotZ(a1);
        rx.SetFromRotX(a2);
        EXVector pos = m_pMatrix[i].r[3];
        m_pMatrix[i].r[3] = EXVector(0.0f, 0.0f, 0.0f, 1.0f);
        m_pMatrix[i] = m_pMatrix[i] * (rx * rz);
        m_pMatrix[i].r[3] = pos;
    }
}

void EXCluster::GenerateFinalInstanceMatrix(EXGeoCluster *pGeo)
{
    EXMatrix rot;
    rot.SetRotationZXY(EXVector(pGeo->m_rot, 0.0f));
    EXVector3 trans(pGeo->m_pos.x, pGeo->m_pos.y, pGeo->m_pos.z);
    for (int i = 0; i < pGeo->m_count; i++) {
        EXVector v = m_pMatrix[i].r[3];
        EXVector pos = ClusterAddXYZ((const EXMaths::mat_44 &)rot * v, trans);
        m_pMatrix[i].r[3] = EXVector(0.0f, 0.0f, 0.0f, 1.0f);
        m_pMatrix[i] = m_pMatrix[i] * rot;
        m_pMatrix[i].r[3] = pos;
    }
}

EXClusterArray::~EXClusterArray()
{
    if (m_pData) {
        delete[] m_pData;
        m_pData = 0;
        m_count = 0;
    }
}

void EXCluster::GenerateInstance(EXGeoCluster *pGeo)
{
    EXScaleMatrix scale(pGeo->m_sizeX, 1.0f, pGeo->m_sizeZ);
    int i;
    for (i = 0; i < pGeo->m_count; i++) {
        m_pMatrix[i].SetIdentity();
        m_pColour[i].set(1.0f, 1.0f, 1.0f, 1.0f);
        float x, z;
        if (pGeo->m_flags & 1) {
            do {
                x = m_rand.Randf() - 0.5f;
                z = m_rand.Randf() - 0.5f;
                m_pDistance[i] = (z * z + x * x) * 4.0f;
            } while (!(m_pDistance[i] <= 1.0f));
        } else {
            x = m_rand.Randf() - 0.5f;
            z = m_rand.Randf() - 0.5f;
            float zz = z * z;
            float xx = x * x;
            m_pDistance[i] = max(xx, zz) * 4.0f;
        }
        m_pMatrix[i].r[3].x = x;
        m_pMatrix[i].r[3].z = z;
        EXVector v = m_pMatrix[i].r[3];
        EXVector pos = (const EXMaths::mat_44 &)scale * v;
        float r[8];
        for (int j = 0; j < 8; j++) r[j] = m_rand.Randf();
        if (pGeo->m_yawRange > 0.0f) {
            EXMatrix m;
            m.SetFromRotY(-((r[0] * 2.0f - 1.0f) * pGeo->m_yawRange * CLUSTER_DEG2RAD));
            m_pMatrix[i] = m_pMatrix[i] * m;
        }
        if (pGeo->m_tiltRange > 0.0f) {
            EXMatrix m;
            m.SetRotationZXY(EXVector((r[2] * 2.0f - 1.0f) * pGeo->m_tiltRange * CLUSTER_DEG2RAD, 0.0f,
                                      -((r[1] * 2.0f - 1.0f) * pGeo->m_tiltRange * CLUSTER_DEG2RAD), 0.0f));
            m_pMatrix[i] = m_pMatrix[i] * m;
        }
        m_pMatrix[i].r[3].set(pos);
        m_pMatrix[i].r[3].w = 1.0f;
        if (pGeo->m_flags & 0x100) {
            float s = pGeo->m_scale.Evaluate(m_pDistance[i], r[3]);
            EXMatrix m;
            m.SetScale(EXVector(s, s, s, 1.0f));
            m_pMatrix[i] = m_pMatrix[i] * m;
        }
        if (pGeo->m_flags & 0x200) {
            float v = pGeo->m_alpha.Evaluate(m_pDistance[i], r[4]);
            if (v > 1.0f) v = 1.0f;
            else if (v < 0.0f) v = 0.0f;
            m_pColour[i].w = v;
        }
        if (pGeo->m_flags & 0x400) {
            float v = pGeo->m_red.Evaluate(m_pDistance[i], r[5]);
            if (v > 1.0f) v = 1.0f;
            else if (v < 0.0f) v = 0.0f;
            m_pColour[i].x = v;
        }
        if (pGeo->m_flags & 0x800) {
            float v = pGeo->m_green.Evaluate(m_pDistance[i], r[6]);
            if (v > 1.0f) v = 1.0f;
            else if (v < 0.0f) v = 0.0f;
            m_pColour[i].y = v;
        }
        if (pGeo->m_flags & 0x1000) {
            float v = pGeo->m_blue.Evaluate(m_pDistance[i], r[7]);
            if (v > 1.0f) v = 1.0f;
            else if (v < 0.0f) v = 0.0f;
            m_pColour[i].z = v;
        }
    }
    for (u32 k = 0; k < pGeo->m_count; k++) {
        float px = m_pMatrix[k].r[3].x;
        float pz = m_pMatrix[k].r[3].z;
        const float *g = pGeo->m_ground;
        m_pMatrix[k].r[3].y = (g[4] * pz + g[5] * px + g[3]) * pz + (px * g[2] + g[1]) * px + g[0];
    }
}

void EXCluster::BuildPreCalculatedPositions(EXGeoCluster *pGeo)
{
    m_rand.SetSeed(pGeo->m_seed);
    m_pDrawData = new EXGDiData[pGeo->m_count];
    m_pMatrix = new EXMatrix[pGeo->m_count];
    m_pDistance = new float[pGeo->m_count];
    m_pColour = new EXVector[pGeo->m_count];
    GenerateInstance(pGeo);
    GenerateInstanceAlignment(pGeo);
    GenerateFinalInstanceMatrix(pGeo);
    for (int i = 0; i < pGeo->m_count; i++) m_pDrawData[i].m_Flags |= 0x40;
    EXFree(m_pDistance);
    m_pDistance = 0;
}

void EXCluster::Draw(EXGeoHeader *pHdr, EXGeoCluster *pGeo)
{
    if (!pHdr) return;
    if (!m_pMatrix) BuildPreCalculatedPositions(pGeo);
    EXVector rgba = EXBaseRenderEnv::RGBAMult();
    EXRenderInfo saved = EXBaseRenderEnv::RenderInfo();
    EXRenderInfo info = saved;
    EXGeoEntity *e = pHdr->GeoEntity(pGeo->m_entity);
    if (!e) return;
    for (u32 i = 0; i < pGeo->m_count; i++) {
        EXBaseDisplay::SetObjectToWorld(m_pMatrix[i]);
        EXBaseDisplay::GetDisplay()->UpdateTransforms();
        info.SetRGBAMult(m_pColour[i] * rgba);
        info.EnableRGBMult((pGeo->m_flags & 0x1c00) != 0);
        info.EnableAlphaMult((pGeo->m_flags >> 9) & 1);
        EXBaseRenderEnv::SetRenderInfo(info);
        e->Draw(&m_pDrawData[i], 0);
    }
    EXBaseRenderEnv::SetRenderInfo(saved);
}
