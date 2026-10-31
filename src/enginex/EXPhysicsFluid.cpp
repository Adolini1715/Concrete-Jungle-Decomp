#include "EXPFluid.h"
#include "EXItem.h"
#include "EXItemAnimator_Entity.h"
#include "EXGeoComplexRefObject.h"
#include "EXGeoHeader.h"
#include "EXGeoEntity.h"
#include "EXPhysicsStandIn.h"
#include "EXMemory.h"


// TODO layout: EXGeoEntity (geo area) keeps a relptr to its source vertices at +0x5c.
static inline EXGeoEntity_VtxData *EntitySourceVerts(EXGeoEntity *pEnt)
{
    return *(ex::EXRelPtr<EXGeoEntity_VtxData> *)((u8 *)pEnt + 0x5c);
}

EX_IMPLEMENT_DYNCREATE(EXPFluid, EXItemPhysicsExt)

s8 EXPFluid::Initialise() { return 1; }

EXPFluid::~EXPFluid()
{
    if (m_pNormals) delete[] m_pNormals;
    if (m_pBuffer[0]) EXFree(m_pBuffer[0]);
    if (m_pBuffer[1]) EXFree(m_pBuffer[1]);
}

// NEAR: 78 bytes differ, retail folds the x-1/x+1 into the index (lea) instead of the displacement.
s8 EXPFluid::Hit(int x, int y, float depth, float spread) const
{
    if (!m_bInitialised) return 0;
    if (x >= m_width - 1 || y >= m_height - 1 || x <= 0 || y <= 0) return 0;
    Cell(x, y) -= depth;
    if (y > 1) {
        if (x > 1) Cell(x - 1, y - 1) -= depth * spread;
        Cell(x, y - 1) -= depth * spread;
        if (x < m_width - 2) Cell(x + 1, y - 1) -= depth * spread;
    }
    if (x > 1) Cell(x - 1, y) -= depth * spread;
    if (x < m_width - 2) Cell(x + 1, y) -= depth * spread;
    if (y < m_height - 2) {
        if (x > 1) Cell(x - 1, y + 1) -= depth * spread;
        Cell(x, y + 1) -= depth * spread;
        if (x < m_width - 2) Cell(x + 1, y + 1) -= depth * spread;
    }
    return 1;
}

EXPFluid::EXPFluid()
{
    m_time = 0.0f;
    m_bInitialised = 0;
    m_bNormals = 0;
    m_pBuffer[0] = 0;
    m_pBuffer[1] = 0;
    m_pNormals = 0;
    m_speed = 1.0f;
}

s8 EXPFluid::Initialise(u32 w, u32 h, float d, float c, float mu, s8 bNormals)
{
    float t = 1.0f / ((float)EXBaseApp::NominalGameFrameRate() * m_speed);
    m_width = w;
    m_spacing = d;
    m_height = h;
    u32 n = w * h;
    m_pBuffer[0] = (float *)_EXAlloc(n * sizeof(float), 0);
    m_pBuffer[1] = (float *)_EXAlloc(n * sizeof(float), 0);
    if (bNormals) m_pNormals = new EXVector3[n];
    float f1 = t * t * c * c / (d * d);
    float f = 1.0f / (t * mu + 2.0f);
    m_k1 = (4.0f - f1 * 8.0f) * f;
    m_k2 = (t * mu - 2.0f) * f;
    m_k3 = f * f1 * 2.0f;
    EXItem *pItem = m_pItem;
    if (!pItem->GeoRefObject().m_pGeoFile) return 0;
    if (!*(void **)((u8 *)pItem->GeoRefObject().m_pGeoFile + 0x20)) return 0;  // TODO layout: EXGeoFile header pointer
    EXGeoEntityHeader *pHdr = pItem->GeoRefObject().GeoEntityHeader();
    if (!pHdr) return 0;
    EXGeoEntity *pEnt = pHdr->m_Addr[0].m_pObject;
    if (!pEnt) return 0;
    EXGeoEntity_VtxData *pVerts = EntitySourceVerts(pEnt);
    if (pEnt->GetVertexCount() != n) return 0;
    m_current = 0;
    u32 i = 0;
    for (u32 y = 0; y < h; y++) {
        for (u32 x = 0; x < w; x++) {
            m_pBuffer[0][i] = pVerts->m_y;
            m_pBuffer[1][i] = m_pBuffer[0][i];
            if (bNormals) m_pNormals[i].Set(0.0f, 1.0f, 0.0f);
            i++;
        }
    }
    m_bNormals = bNormals;
    m_bInitialised = 1;
    return 1;
}

void EXPFluid::SyncWithVerts() const
{
    if (!m_bInitialised) return;
    EXItem *pItem = m_pItem;
    EXGDiData *pGDi = (EXGDiData *)((EXItemAnimator_Entity *)pItem->GetAnimator())->GDiDataArray(0).m_00;
    EXGeoEntityHeader *pHdr = pItem->GeoRefObject().GeoEntityHeader();
    if (!pHdr) return;
    EXGeoEntity *pEnt = pHdr->m_Addr[0].m_pObject;
    if (!pEnt) return;
    if (!pGDi) return;
    EXGeoEntity_VtxData *pVerts = (EXGeoEntity_VtxData *)pEnt->GetVertexBuffer(pGDi, 1);
    if (!pVerts) return;
    u32 n = m_height * m_width;
    for (u32 i = 0; i < n; i++) {
        pVerts[i].m_y = m_pBuffer[m_current][i];
        if (m_bNormals) {
            const EXVector3 &nm = m_pNormals[i];
            pVerts[i].m_Normal = ((((int)(nm.z * 511.0f) << 11) | ((int)(nm.y * 1023.0f) & 0x7ff)) << 11) | ((int)(nm.x * 1023.0f) & 0x7ff);
        }
    }
    pEnt->ReleaseVertexBuffer(pGDi);
}

// NEAR: 80 bytes differ, the inlined Normalise multiplies z as inv*z (retail z*inv, keeping inv live).
s8 EXPFluid::DoUpdate()
{
    if (!m_bInitialised) return 0;
    if (!(m_physicsFlags & 0x20)) {
        float tmp81 = EXBaseApp::GameFrameRateMul();
m_time += m_speed * tmp81;
        while (m_time >= 1.0f) {
            m_time -= 1.0f;
            for (int y = 1; y < m_height - 1; y++) {
                float *pSrc = m_pBuffer[m_current] + y * m_width;
                float *pDst = m_pBuffer[1 - m_current] + y * m_width;
                for (int x = 1; x < m_width - 1; x++) {
                    pDst[x] = (pSrc[x - m_width] + pSrc[x + m_width] + pSrc[x - 1] + pSrc[x + 1]) * m_k3 +
                              pSrc[x] * m_k1 + m_k2 * pDst[x];
                }
            }
            m_current = 1 - m_current;
            if (m_bNormals) {
                for (int y = 1; y < m_height - 1; y++) {
                    float *pBuf = m_pBuffer[m_current] + y * m_width;
                    EXVector3 *pNrm = m_pNormals + y * m_width;
                    for (int x = 1; x < m_width - 1; x++) {
                        pNrm[x].x = pBuf[x - 1] - pBuf[x + 1];
                        pNrm[x].z = pBuf[x - m_width] - pBuf[x + m_width];
                        pNrm[x].y = m_spacing * 2.0f;
                        pNrm[x].Normalise();
                    }
                }
            }
        }
    }
    SyncWithVerts();
    return 1;
}
