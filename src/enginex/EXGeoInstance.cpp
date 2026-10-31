#include "EXGeoInstance.h"
#include "EXGeoEntity.h"
#include "EXGeoFile.h"
#include "XBGeoDrawInfo.h"
#include "EXMemory.h"
#include "EXStdFuncs.h"

EXInstanceEnv::~EXInstanceEnv()
{
    if (m_pBuffer) {
        EXFree(m_pBuffer);
        m_pBuffer = 0;
    }
    m_Count = 0;
}

void EXInstanceManager::ReduceInstanceBuffer(unsigned int n) { m_pCurrent->m_Count -= n; }

void EXInstanceManager::SetParent(const EXMatrix &m)
{
    if (m_pCurrent) {
        m_pCurrent->m_Parent = m;
        m_pCurrent->m_HasParent = 1;
    }
}

EXInstanceEnv::EXInstanceEnv()
{
    m_Hash = 0;
    m_pHeader = 0;
    m_Count = 0;
    m_pBuffer = 0;
    m_pGDi = 0;
}

void EXInstanceManager::DeleteManager()
{
    for (int i = 0; i < m_Envs.Size(); i++) {
        if (m_Envs[i].m_pBuffer) {
            EXFree(m_Envs[i].m_pBuffer);
            m_Envs[i].m_pBuffer = 0;
        }
        if (m_Envs[i].m_pGDi) {
            delete m_Envs[i].m_pGDi;
            m_Envs[i].m_pGDi = 0;
        }
    }
    m_Envs.Empty(1);
    m_Buffers.Empty(1);
    m_NumEnvs = 0;
    m_NumBuffers = 0;
    m_MaxEnvs = 0;
    m_MaxBuffers = 0;
    m_pCurrent = 0;
}

signed char EXInstanceManager::AddInstanceBuffer(EXInstanceEnv *pEnv)
{
    m_Buffers[(int)m_NumBuffers] = pEnv;
    if (++m_NumBuffers >= m_MaxBuffers) {
        m_Buffers.SetGrowBy(32);
        m_MaxBuffers += 32;
    }
    return 1;
}

// NEAR: retail keeps 0 in ebp for all three loops and spills the env offset; logic matches.
void EXInstanceManager::RenderInstances()
{
    for (u32 i = 0; i < m_NumEnvs; i++) {
        EXInstanceEnv *pEnv = &m_Envs[i];
        if (pEnv->m_Count > 0) {
            EXGeoHeader *pHdr = pEnv->m_pHeader->GeoFile()->GeoHeader();
            ((EXGeoInstanceEntity *)pHdr->GeoEntity(pEnv->m_Hash))->Render(pEnv);
        }
        pEnv->m_Count = 0;
        pEnv->m_Hash = 0;
        pEnv->m_pHeader = 0;
    }
    m_NumEnvs = 0;
    for (u32 j = 0; j < m_MaxEnvs; j++) {
        EXInstanceEnv *pEnv = &m_Envs[j];
        if (pEnv->m_Life > 0) {
            if (--pEnv->m_Life == 0) {
                if (pEnv->m_pBuffer) {
                    EXFree(pEnv->m_pBuffer);
                    pEnv->m_pBuffer = 0;
                }
                if (pEnv->m_pGDi) {
                    delete pEnv->m_pGDi;
                    pEnv->m_pGDi = 0;
                }
            }
        }
    }
    for (u32 k = 0; k < m_NumBuffers; k++) {
        EXInstanceEnv *pEnv = m_Buffers[(int)k];
        if (pEnv->m_Count > 0) ((EXGeoInstanceEntity *)pEnv->m_pHeader->GeoEntity(pEnv->m_Hash))->Render(pEnv);
    }
    m_NumBuffers = 0;
}

void EXInstanceManager::ResetInstanceManager()
{
    u32 i;
    for (i = 0; i < m_NumEnvs; i++) m_Envs[i].m_Count = 0;
    for (i = 0; i < m_NumBuffers; i++) m_Buffers[(int)i] = 0;
    m_NumEnvs = 0;
    m_NumBuffers = 0;
    m_pCurrent = 0;
}

EXInstanceManager::~EXInstanceManager() { DeleteManager(); }

signed char EXInstanceManager::AllocateManager()
{
    m_NumEnvs = 0;
    m_NumBuffers = 0;
    m_MaxEnvs = 64;
    m_MaxBuffers = 64;
    m_Envs.SetSize(m_MaxEnvs, -1);
    m_Buffers.SetSize(m_MaxBuffers, -1);
    u32 i;
    for (i = 0; i < m_MaxBuffers; i++) {
        m_Envs[i].m_pHeader = 0;
        m_Envs[i].m_pBuffer = 0;
        m_Envs[i].m_pGDi = 0;
        m_Envs[i].m_Life = 0;
    }
    for (i = 0; i < m_MaxEnvs; i++) m_Buffers[(int)i] = 0;
    m_pCurrent = 0;
    return 1;
}

// NEAR: hash/header registers swapped (ebx/ebp) and the loop-hit return block placed later.
EXInstanceInfo *EXInstanceManager::Instance(EXGeoHeader *pHdr, unsigned int hash, unsigned int key)
{
    if (!m_MaxEnvs) AllocateManager();
    if (m_NumEnvs > 0 && m_pCurrent && m_pCurrent->m_Hash == hash && m_pCurrent->m_pHeader == pHdr && m_pCurrent->m_Key == key
        && !m_pCurrent->m_HasParent && m_pCurrent->m_Count < 200)
        return &m_pCurrent->m_pBuffer[m_pCurrent->m_Count++];
    EXInstanceEnv *pEnv;
    for (u32 i = 0; i < m_NumEnvs; i++) {
        pEnv = &m_Envs[i];
        if (pEnv->m_Hash == hash && pEnv->m_pHeader == pHdr && pEnv->m_Key == key && !pEnv->m_HasParent && pEnv->m_Count < 200) {
            EXInstanceInfo *p = &pEnv->m_pBuffer[pEnv->m_Count];
            m_pCurrent = pEnv;
            pEnv->m_Count++;
            return p;
        }
    }
    if (m_NumEnvs >= m_MaxEnvs) return 0;
    pEnv = &m_Envs[m_NumEnvs];
    if (!pEnv->m_pBuffer) {
        pEnv->m_pBuffer = (EXInstanceInfo *)_EXAlloc(200 * sizeof(EXInstanceInfo), 0);
        if (!pEnv->m_pBuffer) return 0;
    }
    if (pEnv->m_pGDi && (pEnv->m_pGDi->m_10 != hash || pEnv->m_pGDi->m_14 != (u32)pHdr->GeoFile())) {
        delete pEnv->m_pGDi;
        pEnv->m_pGDi = 0;
    }
    if (!pEnv->m_pGDi) {
        pEnv->m_pGDi = new EXGDiInstanceData;
        pEnv->m_pGDi->m_14 = (u32)pHdr->GeoFile();
        pEnv->m_pGDi->m_10 = hash;
    }
    pEnv->m_Key = key;
    pEnv->m_Hash = hash;
    pEnv->m_Count = 0;
    pEnv->m_pHeader = pHdr;
    pEnv->m_Life = 32;
    m_pCurrent = pEnv;
    m_NumEnvs++;
    return &pEnv->m_pBuffer[pEnv->m_Count++];
}

// NEAR: same register/block-order differences as Instance().
EXInstanceInfo *EXInstanceManager::AllocInstanceBuffer(EXGeoHeader *pHdr, unsigned int hash, unsigned int key, unsigned int count)
{
    if (!m_MaxEnvs) AllocateManager();
    if (m_NumEnvs > 0 && m_pCurrent && m_pCurrent->m_Hash == hash && m_pCurrent->m_pHeader == pHdr && m_pCurrent->m_Key == key
        && !m_pCurrent->m_HasParent && m_pCurrent->m_Count + count < 200) {
        EXInstanceInfo *p = &m_pCurrent->m_pBuffer[m_pCurrent->m_Count];
        m_pCurrent->m_Count += count;
        return p;
    }
    EXInstanceEnv *pEnv;
    for (u32 i = 0; i < m_NumEnvs; i++) {
        pEnv = &m_Envs[i];
        if (pEnv->m_Hash == hash && pEnv->m_pHeader == pHdr && pEnv->m_Key == key && !pEnv->m_HasParent
            && pEnv->m_Count + count < 200) {
            EXInstanceInfo *p = &pEnv->m_pBuffer[pEnv->m_Count];
            m_pCurrent = pEnv;
            pEnv->m_Count += count;
            return p;
        }
    }
    if (m_NumEnvs >= m_MaxEnvs) return 0;
    pEnv = &m_Envs[m_NumEnvs];
    if (!pEnv->m_pBuffer) {
        pEnv->m_pBuffer = (EXInstanceInfo *)_EXAlloc(200 * sizeof(EXInstanceInfo), 0);
        memset(pEnv->m_pBuffer, 0, 200 * sizeof(EXInstanceInfo));
        if (!pEnv->m_pBuffer) return 0;
    }
    if (pEnv->m_pGDi && (pEnv->m_pGDi->m_10 != hash || pEnv->m_pGDi->m_14 != (u32)pHdr->GeoFile())) {
        delete pEnv->m_pGDi;
        pEnv->m_pGDi = 0;
    }
    if (!pEnv->m_pGDi) {
        pEnv->m_pGDi = new EXGDiInstanceData;
        pEnv->m_pGDi->m_14 = (u32)pHdr->GeoFile();
        pEnv->m_pGDi->m_10 = hash;
    }
    pEnv->m_pHeader = pHdr;
    pEnv->m_Count = count;
    pEnv->m_Hash = hash;
    pEnv->m_Life = 32;
    pEnv->m_Key = key;
    m_pCurrent = pEnv;
    m_NumEnvs++;
    return pEnv->m_pBuffer;
}

EXInstanceManager::EXInstanceManager() { AllocateManager(); }
