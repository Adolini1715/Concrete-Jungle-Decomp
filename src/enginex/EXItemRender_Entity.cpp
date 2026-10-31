#include "EXItemRender_Entity.h"
#include "EXRenderInfo.h"
#include "EXStdFuncs.h"

// TODO stand-ins: geo-file structures (EXGeoFile / EXGeoEntity area). Offsets in the file are
// self-relative (0 = none).
class EXGeoBaseEntity {
public:
    u8 pad[0x3c];
    float m_3c;             // +0x3c (bounding radius?)
    u8 pad40[0x10];
    u16 m_gdiCount;         // +0x50
    u16 GDiCount()
    {
        if (m_gdiCount) return m_gdiCount;
        return CalcGDiCount();
    }
protected:
    u16 CalcGDiCount();
};
class EXGeoEntityHeader {
public:
    u8 pad[0xc];
    EXGeoBaseEntity *m_pEntity;     // +0xc
    s32 m_lodOff;                   // +0x10 (lod table offset << 8, relative to this field)
    const EXGeoEntityLod *Lods() const
    {
        s32 off = m_lodOff >> 8;
        return off ? (const EXGeoEntityLod *)((const char *)&m_lodOff + off) : 0;
    }
};
class EXGeoEntityLod {
public:
    u8 LodFlags() const { return m_lodFlags; }
    u8 AlphaLayers() const { return m_alphaLayers; }
    float LodDistance(int i) const { return m_dist[i]; }
    EXGeoEntityHeader *EntityHeader() const
    {
        if (!m_hdrOff) return 0;
        return (EXGeoEntityHeader *)((char *)this + m_hdrOff);
    }
    s32 m_hdrOff;           // +0
    float m_dist[4];        // +4
    u16 m_14;
    u8 m_lodFlags;          // +0x16
    u8 m_alphaLayers;       // +0x17
};
struct EXGeoCommonArray {
    u32 m_count;
    s32 m_off;              // +4 relative to this field
};
class EXGeoHeader {
public:
    u8 pad[0x50];
    EXGeoCommonArray m_entities;    // +0x50 (EXGeoEntityHeader, 0x14 bytes each)
};
struct EXLodInfo {
    const EXGeoEntityHeader *m_pHdr;    // +0
    u16 m_lod;                          // +4
    u16 m_flags;                        // +6
    float m_morph;                      // +8
    float m_0c;                         // +0xc
    float m_10;                         // +0x10
    const EXGeoEntityHeader *m_pHdr2;   // +0x14
    u16 m_lod2;                         // +0x18
    u16 m_1a;                           // +0x1a
};
int EXGeoCommonArray_FindIndex(u32 hash, void *pArray, u32 size);

// TODO stand-in (render area).
class EXBaseRenderEnv {
public:
    static s8 LodDisabled() { return ms_RenderInfo.FlagBits().m_lodDisabled; }
protected:
    static EXRenderInfo ms_RenderInfo;
};

void EXItemRender_Entity::RemoveGDiMemory()
{
    for (int i = 0; i < 4; i++) m_gdi[i].RemoveAll();
}

s8 EXItemRender_Entity::Initialise(const EXGeoHeader *pHdr, u32 hash)
{
    const EXGeoCommonArray *pArr = &pHdr->m_entities;
    int idx = EXGeoCommonArray_FindIndex(hash, (void *)pArr, 0x14);
    const EXGeoEntityHeader *pEnt = 0;
    if (idx >= 0) {
        const char *pBase = (const char *)pArr->m_off;
        if (pBase) pBase = (const char *)&pArr->m_off + pArr->m_off;
        pEnt = (const EXGeoEntityHeader *)(pBase + idx * 0x14);
    }
    if (!pEnt) return 0;
    int i;
    const EXGeoEntityLod *pLod = pEnt->Lods();
    if (!pLod) {
        EXGeoBaseEntity *pEntity = pEnt->m_pEntity;
        if (!pEntity) return 0;
        m_gdi[0].Init(pEntity->GDiCount());
    } else {
        for (i = 0;;) {
            EXGeoEntityHeader *pH = pLod->EntityHeader();
            if (!pH) break;
            EXGeoBaseEntity *pEntity = pH->m_pEntity;
            if (!pEntity) return 0;
            m_gdi[i++].Init(pEntity->GDiCount());
            if (i >= 4) break;
            pLod++;
        }
    }
    return 1;
}

// NEAR: 7 bytes, register choice for the min() result pointer.
void EXItemRender_Entity::CalcEntityGeoMorph(EXLodInfo *pInfo, const EXGeoEntityLod *pLod, float dist)
{
    float a = pLod->LodDistance(0);
    float b = pLod->LodDistance(2);
    float t;
    if (a > b) t = (dist - b) / (a - b);
    else t = (dist - a) / (b - a);
    if (t > 0.0f) {
        float m = min(t, 0.99999988f);
        pInfo->m_morph = m;
pInfo->m_flags |= 4;
    }
}

// NEAR: one instruction pair (bias + LodDistance(3)) is computed as movaps/addss mem instead of
// movss mem/addss; the 1-byte shift accounts for the rest.
u32 EXItemRender_Entity::CalcEntityAlphaLod(EXLodInfo *pInfo, const EXGeoEntityLod *pLod, const EXGeoEntityHeader *pHdr, u32 minLod, u32 maxLod, float dist)
{
    float bias = 0.0f;
    if (pHdr && pHdr->m_pEntity) bias = pHdr->m_pEntity->m_3c;
    u32 lod;
    for (lod = 0; lod < maxLod; lod++, pLod++) {
        pHdr = pLod->EntityHeader();
        if (!pHdr) break;
        if (lod >= minLod) {
            u32 f = pLod->LodFlags();
            float d;
            if (f & 0x40) d = pLod->LodDistance(0);
            else if (f & 8) d = bias + pLod->LodDistance(1);
            else if (f & 2) d = pLod->LodDistance(0) - bias;
            else d = pLod->LodDistance(0);
            if (d == 0.0f) break;
            if (d > dist) break;
            if (!pLod[1].EntityHeader() && (f & 8)) return 0;
        }
    }
    pInfo->m_lod = (u16)lod;
    pInfo->m_flags = 0;
    pInfo->m_pHdr = pHdr;
    u32 result = 1;
    if (pLod->LodFlags() & 0x10) {
        const float &d3 = pLod->LodDistance(3);
        if (d3 + bias > dist) {
            pInfo->m_flags = 3;
            pInfo->m_10 = 1.0f;
            pInfo->m_0c = pLod->m_dist[3];
        }
    }
    u8 f = pLod->LodFlags();
    if (f & 0x40) {
        if (dist > pLod->LodDistance(1) - bias) {
            pInfo->m_flags = (u16)((pLod->AlphaLayers() << 8) | 1);
            pInfo->m_10 = pLod->LodDistance(0) - bias;
            float l1 = pLod->LodDistance(1);
            pInfo->m_0c = min(pInfo->m_10, l1);
        }
    } else if (f & 8) {
        if (dist > pLod->LodDistance(0) - bias) {
            pInfo->m_0c = pLod->m_dist[0];
            pInfo->m_10 = pLod->m_dist[1];
            if (pLod[1].EntityHeader()) {
                pInfo->m_flags = 3;
                pInfo->m_1a = 0;
                pInfo->m_pHdr2 = pLod[1].EntityHeader();
                pInfo->m_lod2 = (u16)(lod + 1);
                result = 2;
            } else {
                pInfo->m_flags = 1;
            }
        }
    }
    if (pLod->LodFlags() & 4) CalcEntityGeoMorph(pInfo, pLod, dist);
    return result;
}

u32 EXItemRender_Entity::CalcEntityLod(EXLodInfo *pInfo, const EXGeoEntityHeader *pHdr, u32 minLod, u32 maxLod, float dist)
{
    u32 lod = 0;
    pInfo->m_flags = 0;
    const EXGeoEntityLod *pLod = pHdr->Lods();
    if (pLod && !EXBaseRenderEnv::LodDisabled()) {
        if (pLod->LodFlags() & 0x11) return CalcEntityAlphaLod(pInfo, pLod, pHdr, minLod, maxLod, dist);
        for (;;) {
            pHdr = pLod->EntityHeader();
            if (!pHdr) break;
            if (lod >= maxLod) break;
            if (lod >= minLod) {
                float d = pLod->LodDistance(0);
                if (d == 0.0f) break;
                if (d > dist) break;
            }
            lod++;
            pLod++;
        }
        if (pLod->LodFlags() & 4) CalcEntityGeoMorph(pInfo, pLod, dist);
    }
    pInfo->m_lod = (u16)lod;
    pInfo->m_pHdr = pHdr;
    return 1;
}
