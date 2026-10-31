#include "XBGeoDrawInfo.h"
#include "EXMemory.h"
#include "EXDisplay.h"
#include "EXStdFuncs.h"
#include "EXGeoEntity.h"

extern "C" unsigned long __stdcall D3DResource_Release(void *pResource);

EXD3DResource::EXD3DResource() { ((EXDisplay *)EXBaseDisplay::GetDisplay())->AddD3DResource(this); }

EXD3DResource::~EXD3DResource() { ((EXDisplay *)EXBaseDisplay::GetDisplay())->RemD3DResource(this); }

EXGDiData::EXGDiData() { Clear(); }

void EXGDiData::Init()
{
    m_Flags = 0;
    m_IndexBuffer = 0;
    m_VertexBuffer = 0;
    m_pTexRemap = 0;
    m_64 = 0;
}

void EXGDiData::ReleaseD3DResource() {}

void EXGDiData::Release()
{
    if (m_54) EXFree(m_54);
    if (m_58) EXFree(m_58);
    if (m_5c) EXFree(m_5c);
    if (m_1c) EXGeoEntity::ReleasePhysicalVertexBuffer(m_1c);
    if (m_20) EXGeoEntity::ReleasePhysicalVertexBuffer(m_20);
    if (m_24) EXGeoEntity::ReleasePhysicalVertexBuffer(m_24);
    if (m_pTexRemap) {
        EXFree(m_pTexRemap);
        m_pTexRemap = 0;
    }
    m_64 = 0;
    m_Flags = 0;
    m_IndexBuffer = 0;
    m_VertexBuffer = 0;
    m_54 = 0;
    m_58 = 0;
    m_5c = 0;
    m_1c = 0;
    m_20 = 0;
    m_24 = 0;
    m_CurrentVB = 0;
}

void EXGDiDataArray::Init(int n)
{
    m_pData = new EXGDiData[n];
    m_Size = n;
}

void EXGDiDataArray::RemoveAll()
{
    if (m_pData) {
        for (int i = 0; i < m_Size; i++) {
            EXGDiData *p = &m_pData[i];
            p->ReleaseD3DResource();
            p->Release();
        }
        delete[] m_pData;
        m_pData = 0;
        m_Size = 0;
    }
}

void EXGDiSkinData::Release() {}

void EXGDiSkinDataArray::Init(int n)
{
    m_pData = new EXGDiData[n];
    m_pSkinData = new EXGDiSkinData[n];
    if (m_pSkinData) memset(m_pSkinData, 0, n * sizeof(EXGDiSkinData));
    m_Size = n;
}

void EXGDiSkinDataArray::RemoveAll()
{
    if (m_pData) {
        for (int i = 0; i < m_Size; i++) m_pData[i].Release();
        delete[] m_pData;
        m_pData = 0;
    }
    if (m_pSkinData) {
        delete[] m_pSkinData;
        m_pSkinData = 0;
    }
    m_Size = 0;
}

EXGDiInstanceData::EXGDiInstanceData() { m_pResource = 0; }

EXGDiInstanceData::~EXGDiInstanceData() {}

void EXGDiInstanceData::Release() { ReleaseD3DResource(); }

void EXGDiInstanceData::ReleaseD3DResource()
{
    if (m_pResource) {
        D3DResource_Release(m_pResource);
        m_pResource = 0;
    }
}

EXGDiDataArray::~EXGDiDataArray() { RemoveAll(); }

EXGDiSkinDataArray::~EXGDiSkinDataArray() { RemoveAll(); }
