#include "EXGeoSpreadSheet.h"
#include "EXGeoFile.h"

unsigned int EXGeoSpreadSheet::DataSheetSize(unsigned int i) const
{
    return ((EXGeoDataSheet *)m_DataSheets[i])->m_Size;
}

void *EXGeoSpreadSheet::DataSheetData(unsigned int i) const
{
    return ((EXGeoDataSheet *)m_DataSheets[i])->m_Data;
}

// NEAR: 9 bytes differ, register choice for the section pointer / count (edx vs esi).
// Retail also emits EXGeoCommonArray<EXGeoRefPointerHeader>::operator[]<int> here.
EXGeoTextItem *EXGeoSpreadSheet::FindTextItem(unsigned int hash, const EXGeoFile *pFile) const
{
    if (pFile && pFile->GeoHeader()) {
        const EXGeoRefPointerHeader *pRefs = pFile->GeoHeader()->RefPointerList().Data();
        for (u32 i = 0; i < m_Count; i++) {
            EXGeoTextSection *pSec = (EXGeoTextSection *)pRefs[TextEntries()[i].m_RefIndex].Object(0);
            if (pSec) {
                int lo = 0;
                int hi = pSec->m_Count - 1;
                int r = 1;
EXGeoTextItem *pItems = pSec->m_Items;
                for (;;) {
                    int mid = (r + hi + lo) / 2;
                    if (pItems[mid].m_HashCode == hash) return &pItems[mid];
                    if (mid == lo) break;
                    if (pItems[mid].m_HashCode > hash) {
                        hi = mid;
                        r = 0;
                    } else {
                        lo = mid;
                        r = 1;
                    }
                }
            }
        }
    }
    return 0;
}
