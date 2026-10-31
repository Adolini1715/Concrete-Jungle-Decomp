#include "EXGeoCommonObject.h"
#include "EXGeoHeader.h"
#include "EXGeoFile.h"
#include "EXGeoEntity.h"
#include "EXGeoAnim.h"
#include "EXGeoScript.h"
#include "EXGeoMap.h"
#include "EXGeoParticle.h"
#include "EXGeoFace.h"
#include "EXMemory.h"
#include "EXNew.h"

class EXGeoBaseDebug;
static EXBoundsBox s_NullBoundsBox;  // 0x49e530 (file static)

// Debug file header: per-object debug records then error strings, all relptrs.
class EXGeoDebugHeader {
public:
    EXGeoBaseDebug *GetDebug(unsigned int i) const;
    const char *GetError(unsigned int i) const;
    u32 m_NumDebug;                                 // 0x00
    u32 m_04;
    s32 m_ExtHeader;                                // 0x08
    ex::EXRelPtr<EXGeoBaseDebug> m_Debug[1];        // 0x0c then error strings
};

class EXGeoDebugStats {
public:
    EXGeoDebugStats();
    void Initialise();
    u32 m_00;
    u32 m_04;
};

EX_IMPLEMENT_DYNAMIC_ROOT(EXGeoCommonObject)
EX_IMPLEMENT_DYNAMIC(EXGeoBaseAnim, EXGeoCommonObject)
EX_IMPLEMENT_DYNAMIC(EXGeoAnim, EXGeoBaseAnim)
EX_IMPLEMENT_DYNAMIC(EXGeoBaseAnimSkin, EXGeoCommonObject)
EX_IMPLEMENT_DYNAMIC(EXGeoAnimSkin, EXGeoBaseAnimSkin)
EX_IMPLEMENT_DYNAMIC(EXGeoScript, EXGeoCommonObject)
EX_IMPLEMENT_DYNAMIC(EXGeoMap, EXGeoCommonObject)
EX_IMPLEMENT_DYNAMIC(EXGeoBaseEntity, EXGeoCommonObject)
EX_IMPLEMENT_DYNAMIC(EXGeoEntity, EXGeoBaseEntity)
EX_IMPLEMENT_DYNAMIC(EXGeoSplitEntity, EXGeoBaseEntity)
EX_IMPLEMENT_DYNAMIC(EXGeoInstanceEntity, EXGeoBaseEntity)
EX_IMPLEMENT_DYNAMIC(EXGeoNavMeshEntity, EXGeoBaseEntity)
EX_IMPLEMENT_DYNAMIC(EXGeoMapZoneEntity, EXGeoBaseEntity)
EX_IMPLEMENT_DYNAMIC(EXGeoParticle, EXGeoCommonObject)
EX_IMPLEMENT_DYNAMIC(EXGeoFace, EXGeoCommonObject)

void EXGeoCommonObject::OnCreate(EXGeoHeader *, EXGeoObjectHeader *) {}
void EXGeoCommonObject::OnDelete(EXGeoHeader *, EXGeoObjectHeader *) {}
void EXGeoCommonObject::Draw(EXGDiData *, unsigned int) {}
EXBoundsBox &EXGeoCommonObject::GetBoundsBox() { return s_NullBoundsBox; }

EXGeoBaseDebug *EXGeoDebug_Get(const EXGeoHeader *pHdr, unsigned int i) { return pHdr->GetDebug(i); }

void EXGeoDebugStats::Initialise()
{
    m_00 = 0;
    m_04 = 0;
}

_EXGeoCommonObjectDefArray *_EXGeoCommonObjectDef::m_gpDefList;

// NEAR: retail passes p through ecx (not eax) in the fallback call.
void _EXGeoCommonObjectDef::Create(void *p, unsigned int type)
{
    if (m_gpDefList && m_gpDefList->Size()) {
        _EXGEOCOMMONOBJECTDEF *pDefs = m_gpDefList->GetData();
        int lo = 0;
        int hi = m_gpDefList->Size() - 1;
        int mid = (hi + lo + 1) / 2;
        _EXGEOCOMMONOBJECTDEF *pDef = &pDefs[mid];
        while (pDef->m_Type != type) {
            if (mid == lo) {
                pDefs[0].m_pfnCreate(p, 0);
                return;
            }
            int r;
            if (pDef->m_Type > type) {
                hi = mid;
                r = 0;
            } else {
                lo = mid;
                r = 1;
            }
            mid = (r + hi + lo) / 2;
            pDef = &pDefs[mid];
        }
        pDef->m_pfnCreate(p, mid);
        return;
    }
    for (;;) {}
}

void *EXGeoXtraData::GetXtra(unsigned char t) const
{
    u16 types = XtraTypes();
    if (types & (1 << t)) {
        ex::EXRELOFFSET24x8 *p = *(ex::EXRelPtr24<ex::EXRELOFFSET24x8> *)this;
        for (int i = t; i > 0; i--) {
            p += types & 1;
            types >>= 1;
        }
        return ex::EXRelOffset(*p);
    }
    return 0;
}

EXGeoBaseDebug *EXGeoDebugHeader::GetDebug(unsigned int i) const { return m_Debug[i]; }

const char *EXGeoDebugHeader::GetError(unsigned int i) const { return (const char *)(EXGeoBaseDebug *)m_Debug[m_NumDebug + i]; }

EXGeoDebugStats::EXGeoDebugStats()
{
    m_00 = 0;
    m_04 = 0;
}

int EXGeoCommonArray_FindIndex2(u32 hash, void *pArray, u32 size)
{
    EXGeoObjectArray *pA = (EXGeoObjectArray *)pArray;
    int n = pA->HashSize();
    if (n > 3) {
        u8 *pData = (u8 *)pA->Data();
        int lo = 0;
        int hi = n - 1;
        int mid = (hi + lo + 1) / 2;
        u32 h = *(u32 *)(pData + mid * size);
        while (h != hash) {
            if (mid == lo) goto notFound;
            int r;
            if (h > hash) {
                hi = mid;
                r = 0;
            } else {
                lo = mid;
                r = 1;
            }
            mid = (r + hi + lo) / 2;
            h = *(u32 *)(pData + mid * size);
        }
        return mid;
    }
    {
        u8 *pData = (u8 *)pA->Data();
        for (int i = 0; i < n; pData += size, i++)
            if (hash == *(u32 *)pData) return i;
    }
notFound:
    return -1;
}

void EXGeoCommonArray_UpdatePtrs(EXGeoHeader *pHdr, const EXGeoObjectArray *pArray, int size, int nAddr)
{
    EXGeoObjectHeader *pElem = pArray->Data();
    void *pBase = 0;
    const EXGeoSectionHeader *pSections = pHdr->SectionList().Data();
    int lastSec = -1;
    for (int n = pArray->Size(); n > 0; n--) {
        s8 notCreated = !pElem->ObjectCreated();
        u32 sec = pElem->Section();
        if (sec) {
            if (sec != (u32)lastSec) {
                pBase = pHdr->SectionLoadAddr(sec, 1);
                lastSec = sec;
            }
            for (int a = 0; a < nAddr; a++) {
                if (pBase)
                    pElem->Address(a).m_pObject = (EXGeoCommonObject *)ex::EXRelocOffset(pBase, pElem->Address(a).m_Offset, pSections[sec].m_Seek);
                else
                    pElem->Address(a).m_pObject = 0;
            }
            if (!pBase) goto next;
        } else {
            for (int a = 0; a < nAddr; a++)
                pElem->Address(a).m_pObject = (EXGeoCommonObject *)ex::EXAbsOffset(pHdr, pElem->Address(a).m_Offset);
        }
        if (notCreated) {
            pElem->SetObjectCreated();
            pElem->Address(0).m_pObject->Create(pHdr, pElem);
        }
next:
        pElem = (EXGeoObjectHeader *)((u8 *)pElem + size);
    }
}

void EXGeoCommonArray_SetPtrs(const EXGeoHeader *pHdr, const EXGeoObjectArray *pArray, int size)
{
    EXGeoObjectHeader *pElem = pArray->Data();
    void *pBase = 0;
    const EXGeoSectionHeader *pSections = pHdr->SectionList().Data();
    int lastSec = -1;
    for (int n = pArray->Size(); n > 0; n--) {
        u32 sec = pElem->Section();
        if (sec) {
            if (sec != (u32)lastSec) {
                pBase = pHdr->SectionLoadAddr(sec, 1);
                lastSec = sec;
            }
            if (pBase)
                pElem->m_Addr[0].m_pObject = (EXGeoCommonObject *)ex::EXRelocOffset(pBase, pElem->m_Addr[0].m_Offset, *(const ex::EXABSOFFSET32 *)&pSections[sec].m_SeekPos);
            else
                pElem->m_Addr[0].m_pObject = 0;
        } else {
            pElem->m_Addr[0].m_pObject = (EXGeoCommonObject *)ex::EXAbsOffset(pHdr, pElem->m_Addr[0].m_Offset);
        }
        pElem = (EXGeoObjectHeader *)((u8 *)pElem + size);
    }
}

void EXGeoCommonArray_OnDelete(EXGeoHeader *pHdr, const EXGeoObjectArray *pArray, int size, int section)
{
    EXGeoObjectHeader *pElem = pArray->Data();
    void *pBase = 0;
    const EXGeoSectionHeader *pSections = pHdr->SectionList().Data();
    int lastSec = -1;
    for (int i = 0; i < pArray->Size(); i++) {
        if (pElem->ObjectCreated()) {
            u32 sec = pElem->Section();
            if (sec == (u32)section) {
                EXGeoCommonObject *pObj;
                if (sec) {
                    if (sec != (u32)lastSec) {
                        pBase = pHdr->SectionLoadAddr(sec, 0);
                        if (!pBase) for (;;) {}
                        lastSec = sec;
                    }
                    pObj = (EXGeoCommonObject *)ex::EXRelocOffset(pBase, pElem->m_Addr[0].m_Offset, *(const ex::EXABSOFFSET32 *)&pSections[sec].m_SeekPos);
                } else {
                    pObj = (EXGeoCommonObject *)ex::EXAbsOffset(pHdr, pElem->m_Addr[0].m_Offset);
                }
                if (pObj) {
                    pObj->OnDelete(pHdr, pElem);
                    pElem->m_Addr[0].m_pObject = 0;
                    pElem->ClrObjectCreated();
                }
            }
        }
        pElem = (EXGeoObjectHeader *)((u8 *)pElem + size);
    }
}

void EXGeoCommonArray_OnSectionDelete(EXGeoHeader *pHdr, const EXGeoObjectArray *pArray, int size)
{
    EXGeoObjectHeader *pElem = pArray->Data();
    void *pBase = 0;
    s8 objectsDelete = 0;
    const EXGeoSectionHeader *pSections = pHdr->SectionList().Data();
    int lastSec = -1;
    for (int n = pArray->Size(); n > 0; n--) {
        u32 sec = pElem->Section();
        if (sec && pElem->ObjectCreated()) {
            if (sec != (u32)lastSec) {
                EXGeoSectionFile *pFile = pSections[sec].m_pFile;
                lastSec = sec;
                if (pFile) {
                    objectsDelete = pFile->ObjectsDelete();
                    if (!objectsDelete) goto next;
                    pBase = pFile->MemData();
                }
            }
            if (objectsDelete) {
                EXGeoCommonObject *pObj = (EXGeoCommonObject *)ex::EXRelocOffset(pBase, pElem->m_Addr[0].m_Offset, *(const ex::EXABSOFFSET32 *)&pSections[sec].m_SeekPos);
                pObj->OnDelete(pHdr, pElem);
                pElem->m_Addr[0].m_pObject = 0;
                pElem->ClrObjectCreated();
            }
        }
next:
        pElem = (EXGeoObjectHeader *)((u8 *)pElem + size);
    }
}

void EXGeoCommonArray_DeletePtrs(const EXGeoHeader *pHdr, const EXGeoObjectArray *pArray, int size, int section)
{
    EXGeoObjectHeader *pElem = pArray->Data();
    for (int n = pArray->Size(); n > 0; n--) {
        if (pElem->Object(0) && pElem->Section() == (u32)section) pElem->m_Addr[0].m_pObject = 0;
        pElem = (EXGeoObjectHeader *)((u8 *)pElem + size);
    }
}

_EXGeoCommonObjectDef::~_EXGeoCommonObjectDef()
{
    if (m_gpDefList) {
        delete m_gpDefList;
        m_gpDefList = 0;
    }
}

int EXGeoCommonArray_FindIndex(u32 hash, void *pArray, u32 size)
{
    EXGeoObjectArray *pA = (EXGeoObjectArray *)pArray;
    int idx = EXGeoHashIndex(hash);
    if (idx >= 0) {
        if (idx < pA->Size()) return idx;
        return -1;
    }
    int hs = pA->HashSize();
    if (hs > 0) return EXGeoCommonArray_FindIndex2(hash, pArray, size);
    u32 *pHashes = (u32 *)pA->Data() + hs;
    int n = -hs;
    if (n > 3) {
        int lo = 0;
        int hi = n - 1;
        int mid = (hi + lo + 1) / 2;
        u32 h = pHashes[mid];
        while (h != hash) {
            if (mid == lo) goto notFound;
            int r;
            if (h > hash) {
                hi = mid;
                r = 0;
            } else {
                lo = mid;
                r = 1;
            }
            mid = (r + hi + lo) / 2;
            h = pHashes[mid];
        }
        return mid;
    }
    {
        for (int i = 0; i < n; i++)
            if (hash == pHashes[i]) return i;
    }
notFound:
    return -1;
}

_EXGeoCommonObjectDef::_EXGeoCommonObjectDef(_EXGEOCOMMONOBJECTDEF def)
{
    if (!m_gpDefList) m_gpDefList = new _EXGeoCommonObjectDefArray;
    int i;
    for (i = 0; i < m_gpDefList->Size(); i++)
        if (m_gpDefList->ElementAt(i).m_Type > def.m_Type) break;
    m_gpDefList->InsertAt(i, def, 1);
}

// Two dynamic-looking initialisers precede the registrations in retail ($E1/$E2).
static const float s_GeoCommonDummyInit = 0.0f;
static float s_GeoCommonDummy = s_GeoCommonDummyInit;

EX_IMPLEMENT_GEOOBJECT(EXGeoCommonObject, 0x000)
EX_IMPLEMENT_GEOOBJECT(EXGeoBaseAnim, 0x100)
EX_IMPLEMENT_GEOOBJECT(EXGeoAnim, 0x102)
EX_IMPLEMENT_GEOOBJECT(EXGeoBaseAnimSkin, 0x201)
EX_IMPLEMENT_GEOOBJECT(EXGeoAnimSkin, 0x200)
EX_IMPLEMENT_GEOOBJECT(EXGeoScript, 0x300)
EX_IMPLEMENT_GEOOBJECT(EXGeoMap, 0x500)
EX_IMPLEMENT_GEOOBJECT(EXGeoBaseEntity, 0x600)
EX_IMPLEMENT_GEOOBJECT(EXGeoEntity, 0x601)
EX_IMPLEMENT_GEOOBJECT(EXGeoSplitEntity, 0x603)
EX_IMPLEMENT_GEOOBJECT(EXGeoInstanceEntity, 0x606)
EX_IMPLEMENT_GEOOBJECT(EXGeoNavMeshEntity, 0x607)
EX_IMPLEMENT_GEOOBJECT(EXGeoMapZoneEntity, 0x608)
EX_IMPLEMENT_GEOOBJECT(EXGeoParticle, 0x700)
EX_IMPLEMENT_GEOOBJECT(EXGeoFace, 0x800)
