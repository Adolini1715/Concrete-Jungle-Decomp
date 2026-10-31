#ifndef GEOSECTIONMANAGER_H
#define GEOSECTIONMANAGER_H
#include "types.h"
#include "EXList.h"

// A reference-counted request for one geo-file section (sizeof 0x24). Status: 0 idle, 1 loading
// the section, 2 file/section loaded but not compacted, 3 ready.
class GeoSectionItem : public EXDListItem {
public:
    GeoSectionItem(int geoHash, int itemHash);
    ~GeoSectionItem();
    signed char Update(signed char bForce);
    signed char IsLoaded();
    void Invalidate() { m_bValid = 0; }
    signed char Invalid() { return m_bValid == 0; }
    signed char AutoLoadedBase() { return m_bAutoLoadedBase; }
    void IncUsage(int n) { m_Usage += n; }
    void DecUsage(int n)
    {
        m_Usage -= n;
        if (m_Usage < 0) m_Usage = 0;
        if (m_Usage == 0) m_DeleteTimer = 60;
    }
    int GetUsage() { return m_Usage; }
    int GetGeoHash() { return m_GeoHash; }
    int GetSubSection() { return m_SubSection; }
    int GetItemHash() { return m_ItemHash; }
    void DetachFromGameflow(signed char b) { m_bDetached = b; }
    signed char IsDetachedFromGameflow() { return m_bDetached; }
    signed char NeedsDelete()
    {
        if (m_Usage == 0 && m_DeleteTimer-- <= 0) return 1;
        return 0;
    }
    int m_Status;               // 0x08
    int m_GeoHash;              // 0x0c
    int m_ItemHash;             // 0x10
    int m_SubSection;           // 0x14
    int m_Usage;                // 0x18
    int m_DeleteTimer;          // 0x1c frames left once unused
    s8 m_bValid;                // 0x20
    s8 m_bAutoLoadedBase;       // 0x21 the geo file was loaded for this item
    s8 m_bSectionLoaded;        // 0x22 the section was loaded for this item
    s8 m_bDetached;             // 0x23
};

class GeoSectionManager {
public:
    static void Shutdown() { Init(1); }
    static void SetAsyncMode(signed char b) { m_bIsAsync = b; }
    static signed char GetAsyncMode() { return m_bIsAsync; }
    static void DebugWindowToggle() {}
    static void DebugInit() {}
    static void DebugWindowUpdate() {}
    static void DebugInit(signed char) {}
    static void DebugRegisterItem(int, int, GeoSectionItem *) {}
    static void DebugUnregisterItem(int, int) {}
    static void DebugCompact(GeoSectionItem *, GeoSectionItem *) {}
    static void DebugUpdate() {}
    GeoSectionManager();
    ~GeoSectionManager();
    static int GetSectionCount();
    static void DetachFromGameflow(int geoHash, signed char b);
    static GeoSectionItem *Find(int geoHash, GeoSectionItem *pExclude);
    static signed char FileInUse(int geoHash);
    static void Init(signed char bAll);
    static int GetSubSection(int geoHash, int itemHash);
    static signed char Compact(int geoHash, int subSection);
    static GeoSectionItem *Find(int geoHash, int itemHash);
    static void Del(int geoHash, int itemHash);
    static void Update();
    static signed char IsLoaded(int geoHash, int itemHash);
    static GeoSectionItem *Add(int geoHash, int itemHash);
    static EXDList m_Sections;
private:
    friend class GeoSectionItem;
    static signed char m_bIsAsync;
};
#endif
