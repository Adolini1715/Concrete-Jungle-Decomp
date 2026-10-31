#ifndef EXITEMRENDER_ENTITY_H
#define EXITEMRENDER_ENTITY_H
#include "types.h"

class EXGeoHeader;
class EXGeoEntityHeader;
class EXGeoEntityLod;
struct EXLodInfo;

// TODO stand-in: per-LOD GDi data array (XBGeoEntity area); ctor verified (two zeroed dwords).
#ifndef EXGDIDATAARRAY_DEFINED
#define EXGDIDATAARRAY_DEFINED
class EXGDiDataArray {
public:
    EXGDiDataArray() { m_00 = 0; m_04 = 0; }
    ~EXGDiDataArray();
    void Init(int n);
    void RemoveAll();
    int Size() const { return m_04; }  // EXGeoMap.obj (XBGeoDrawInfo.h copy: m_Size)
    u32 m_00;
    u32 m_04;
};
#endif

// Entity renderer state: one GDi data array per LOD (up to 4). Not a runtime class.
class EXItemRender_Entity {
public:
    EXItemRender_Entity() {}
    ~EXItemRender_Entity() {}
    const EXGDiDataArray &GDiDataArray(int i) const { return m_gdi[i]; }
    void RemoveGDiMemory();
    s8 Initialise(const EXGeoHeader *pHdr, u32 hash);
    static u32 CalcEntityLod(EXLodInfo *pInfo, const EXGeoEntityHeader *pHdr, u32 minLod, u32 maxLod, float dist);
private:
    static void CalcEntityGeoMorph(EXLodInfo *pInfo, const EXGeoEntityLod *pLod, float dist);
    static u32 CalcEntityAlphaLod(EXLodInfo *pInfo, const EXGeoEntityLod *pLod, const EXGeoEntityHeader *pHdr, u32 minLod, u32 maxLod, float dist);
public:
    EXGDiDataArray m_gdi[4];
};
#endif
