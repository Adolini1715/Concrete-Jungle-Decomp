#include "GeoSectionManager.h"
#include "EXMemory.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "FEText.h"
#include "XAppStandIn.h"

EXDList GeoSectionManager::m_Sections;
signed char GeoSectionManager::m_bIsAsync = 1;

signed char GeoSectionItem::IsLoaded() { return m_Status == 3; }

GeoSectionManager::GeoSectionManager() { DebugInit(); }
GeoSectionManager::~GeoSectionManager() {}

int GeoSectionManager::GetSectionCount() { return m_Sections.Count(); }

void GeoSectionManager::DetachFromGameflow(int geoHash, signed char b)
{
    for (GeoSectionItem *p = (GeoSectionItem *)m_Sections.m_pHead; p; p = (GeoSectionItem *)p->m_pNext)
        if (p->m_GeoHash == geoHash) p->DetachFromGameflow(b);
}

GeoSectionItem *GeoSectionManager::Find(int geoHash, GeoSectionItem *pExclude)
{
    for (GeoSectionItem *p = (GeoSectionItem *)m_Sections.m_pHead; p; p = (GeoSectionItem *)p->m_pNext)
        if (p != pExclude && p->m_GeoHash == geoHash) return p;
    return 0;
}

signed char GeoSectionManager::FileInUse(int geoHash)
{
    for (GeoSectionItem *p = (GeoSectionItem *)m_Sections.m_pHead; p; p = (GeoSectionItem *)p->m_pNext)
        if (p->m_GeoHash == geoHash) return 1;
    return 0;
}

GeoSectionItem::~GeoSectionItem()
{
    GeoSectionManager::m_Sections.Remove(this);
    GeoSectionManager::DebugUnregisterItem(m_GeoHash, m_ItemHash);
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(m_GeoHash);
    if (m_bAutoLoadedBase && !GeoSectionManager::FileInUse(m_GeoHash)) {
        EXGeoFile::DeLoadGeoFile(m_GeoHash, 1);
        return;
    }
    if (m_bSectionLoaded && m_bValid && pFile && pFile->m_pGeoHeader)
        pFile->m_pGeoHeader->DeLoadSectionFile(m_SubSection);
}

void GeoSectionManager::Init(signed char bAll)
{
    DebugInit(bAll);
    GeoSectionItem *p = (GeoSectionItem *)m_Sections.m_pHead;
    while (p) {
        GeoSectionItem *pNext = (GeoSectionItem *)p->m_pNext;
        if (bAll || !p->IsDetachedFromGameflow()) delete p;
        p = pNext;
    }
}

int GeoSectionManager::GetSubSection(int geoHash, int itemHash)
{
    if (!itemHash) return 0;
    if (itemHash >= 0x8000001 && itemHash <= 0x800007f) return itemHash;
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(geoHash);
    if (!pFile) return 0;
    EXGeoHeader *pHdr = pFile->m_pGeoHeader;
    if (!pHdr) return 0;
    EXGeoObjectHeader *pObj = pHdr->GeoObjectHeader(itemHash);
    if (!pObj) return 0;
    EXGeoSectionHeader *pSec = pHdr->SectionHeader(pObj->m_Flags & 0x7fff);
    if (!pSec) return 0;
    int sec = pSec->m_HashCode;
    if (!sec) return 0;
    if (sec == 0x8000000) return 0;
    return sec;
}

// NEAR: retail keeps subSection in a stack slot and uses ebp for the usage add.
signed char GeoSectionManager::Compact(int geoHash, int subSection)
{
    subSection = GetSubSection(geoHash, subSection);
    signed char r = 0;
    GeoSectionItem *pFirst = 0;
    GeoSectionItem *p = (GeoSectionItem *)m_Sections.m_pHead;
    while (p) {
        GeoSectionItem *pNext = (GeoSectionItem *)p->m_pNext;
        if (p->m_GeoHash == geoHash && p->m_SubSection == subSection) {
            if (!pFirst) pFirst = p;
            else {
                pFirst->m_Usage += p->m_Usage;
                r = 1;
                p->Invalidate();
                DebugCompact(pFirst, p);
            }
        }
        p = pNext;
    }
    return r;
}

GeoSectionItem *GeoSectionManager::Find(int geoHash, int itemHash)
{
    if (!itemHash) return 0;
    int sub = GetSubSection(geoHash, itemHash);
    for (GeoSectionItem *p = (GeoSectionItem *)m_Sections.m_pHead; p; p = (GeoSectionItem *)p->m_pNext) {
        if (p->m_GeoHash == geoHash) {
            if (p->m_SubSection == sub) return p;
            if (p->m_ItemHash == itemHash) return p;
        }
    }
    return 0;
}

signed char GeoSectionItem::Update(signed char bForce)
{
    EXGeoFile *pFile;
    EXGeoHeader *pHdr;
    switch (m_Status) {
    case 0:
        pFile = EXGeoFile::GetGeoFile(m_GeoHash);
        if (!pFile) return 0;
        if (!bForce && !GeoSectionManager::m_bIsAsync && !pFile->m_pGeoHeader) {
            EXGeoFile::DeLoadGeoFile(m_GeoHash, 1);
            pFile = EXGeoFile::LoadGeoFile(m_GeoHash, GeoSectionManager::m_bIsAsync, 0);
        }
        if (!pFile->m_pGeoHeader) return 0;
        m_SubSection = GeoSectionManager::GetSubSection(m_GeoHash, m_ItemHash);
        if (m_SubSection > 0) {
            pHdr = pFile->m_pGeoHeader;
            if (!pHdr) return 0;
            if (pHdr->IsSectionLoaded(m_SubSection, 1) != 3) {
                bForce = 1;
                pHdr->LoadSectionFile(m_SubSection, GeoSectionManager::m_bIsAsync, 0);
                m_Status = 1;
                m_bSectionLoaded = bForce;
            } else {
                m_Status = 2;
            }
        } else {
            m_Status = 2;
            return 0;
        }
    case 1:
        pFile = EXGeoFile::GetGeoFile(m_GeoHash);
        if (!pFile) return 0;
        pHdr = pFile->m_pGeoHeader;
        if (!pHdr) return 0;
        if (!bForce && !GeoSectionManager::m_bIsAsync && pHdr->IsSectionLoaded(m_SubSection, 1) != 3) {
            pHdr->DeLoadSectionFile(m_SubSection);
            pHdr->LoadSectionFile(m_SubSection, GeoSectionManager::m_bIsAsync, 0);
        }
        switch (pHdr->IsSectionLoaded(m_SubSection, 1)) {
        case 0:
        case 5:
            m_Status = 0;
            return 0;
        case 3:
            m_Status = 2;
            break;
        default:
            return 0;
        }
    case 2:
        GeoSectionManager::Compact(m_GeoHash, m_SubSection);
        m_Status = 3;
    case 3:
        return 1;
    }
    return 0;
}

void GeoSectionManager::Del(int geoHash, int itemHash)
{
    GeoSectionItem *p = Find(geoHash, itemHash);
    if (p) p->DecUsage(1);
}

void GeoSectionManager::Update()
{
    DebugUpdate();
    DebugWindowUpdate();
    GeoSectionItem *p = (GeoSectionItem *)m_Sections.m_pHead;
    while (p) {
        GeoSectionItem *pNext = (GeoSectionItem *)p->m_pNext;
        if (p->Invalid()) delete p;
        else if (p->IsDetachedFromGameflow() && p->NeedsDelete()) delete p;
        else if (!p->IsDetachedFromGameflow() && !theApp.GetCutscene() && p->NeedsDelete()) delete p;
        else p->Update(0);
        p = pNext;
    }
}

signed char GeoSectionManager::IsLoaded(int geoHash, int itemHash)
{
    GeoSectionItem *p = Find(geoHash, itemHash);
    if (!p) return 0;
    return p->IsLoaded();
}

GeoSectionItem::GeoSectionItem(int geoHash, int itemHash)
{
    GeoSectionManager::m_Sections.AddTail(this);
    m_Status = 0;
    m_GeoHash = geoHash;
    m_ItemHash = itemHash;
    m_Usage = 1;
    m_SubSection = -1;
    m_bValid = 1;
    m_DeleteTimer = 0;
    m_bAutoLoadedBase = 0;
    m_bSectionLoaded = 0;
    m_bDetached = 0;
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(m_GeoHash);
    if (pFile && pFile->m_pGeoHeader) {
        GeoSectionItem *pOther = GeoSectionManager::Find(geoHash, this);
        if (pOther) {
            if (pOther->m_bAutoLoadedBase) m_bAutoLoadedBase = 1;
        }
    } else {
        EXGeoFile::LoadGeoFile(m_GeoHash, GeoSectionManager::m_bIsAsync, 0);
        m_bAutoLoadedBase = 1;
    }
    Update(1);
}

// NEAR: retail copies the new item into esi before returning (null path xor esi).
GeoSectionItem *GeoSectionManager::Add(int geoHash, int itemHash)
{
    if (!itemHash) return 0;
    GeoSectionItem *p = Find(geoHash, itemHash);
    if (p) {
        p->m_Usage++;
        p->Update(0);
        return p;
    }
    p = new GeoSectionItem(geoHash, itemHash);
    DebugRegisterItem(geoHash, itemHash, p);
    return p;
}
