#ifndef EXGEOSPREADSHEET_H
#define EXGEOSPREADSHEET_H
#include "types.h"
#include "EXRelPtr.h"

class EXGeoFile;

// One translated string entry (0x10): hash, then text data.
class EXGeoTextItem {
public:
    unsigned int HashCode() const { return m_HashCode; }
    u32 m_HashCode;     // +0
    u32 m_04[3];
};
// Text section (a ref-pointer object): sorted EXGeoTextItem table.
struct EXGeoTextSection {
    EXGeoTextItem *Find(u32 hash)
    {
        int lo = 0;
        int hi = m_Count - 1;
        EXGeoTextItem *pItems = m_Items;
        int r = 1;
        for (;;) {
            int mid = (r + hi + lo) / 2;
            if (pItems[mid].m_HashCode == hash) return &pItems[mid];
            if (mid == lo) return 0;
            if (pItems[mid].m_HashCode > hash) {
                hi = mid;
                r = 0;
            } else {
                lo = mid;
                r = 1;
            }
        }
    }
    u32 m_00;
    u32 m_Count;                // +4
    EXGeoTextItem m_Items[1];   // +8
};
// Data sheet: size then raw data.
struct EXGeoDataSheet {
    u32 m_Size;
    u8 m_Data[4];
};
// Text sheet entry: ref-pointer index of a text section.
struct EXGeoTextSheetEntry {
    u32 m_RefIndex;
    u32 m_04;
};

// Spreadsheet object (not a common object): either data sheets or text sections.
class EXGeoSpreadSheet {
public:
    unsigned int DataSheetSize(unsigned int i) const;
    void *DataSheetData(unsigned int i) const;
    EXGeoTextItem *FindTextItem(unsigned int hash, const EXGeoFile *pFile) const;
    u32 m_Count;                                    // +0
    // Data type: relptrs to the sheets from +4. Text type: EXGeoTextSheetEntry table from +8.
    ex::EXRelPtr<EXGeoDataSheet> m_DataSheets[1];   // +4
    const EXGeoTextSheetEntry *TextEntries() const { return (const EXGeoTextSheetEntry *)((char *)this + 8); }
};
#endif
