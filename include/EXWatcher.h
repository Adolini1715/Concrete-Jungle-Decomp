#ifndef EXWATCHER_H
#define EXWATCHER_H
#include "EXDebugTree.h"

namespace ex {
// 32-bit value of any watch type.
union EXVar32 {
    EXVar32() {}
    EXVar32(unsigned int v) { m_u32 = v; }
    EXVar32(int v) { m_s32 = v; }
    EXVar32(float v) { m_f32 = v; }
    unsigned int m_u32;
    int m_s32;
    float m_f32;
};
}

namespace EXWATCH {
// TODO names: 1/4 byte, 2/5 word, 3/6/7/9 dword, 8 signed byte (from RestoreItemValue).
enum DataType { DT_NONE = 0, DT_1, DT_2, DT_3, DT_4, DT_5, DT_6, DT_7, DT_8, DT_9 };
}

// Edit range of a watched item (sizeof 0x10), indexed like the tree items.
struct EXWatchItem {
    EXWATCH::DataType m_Type;  // 0x00
    ex::EXVar32 m_Min;         // 0x04
    ex::EXVar32 m_Max;         // 0x08
    ex::EXVar32 m_Step;        // 0x0c
};

// Debug tree of watched variables (sizeof 0x3c). Item values are addresses.
class EXWatcher : public EXDebugTree {
public:
    virtual ~EXWatcher();
    virtual void DelItem(unsigned int h);
    EXWatcher();
    static EXWatcher *GetWatcher() { return m_pWatcher; }
    static s8 Init(EXWatcher *pWatcher);
    static void Shutdown();
    unsigned int AddItem(const char *pName, unsigned int section, EXWATCH::DataType type, void *pAddr);
    unsigned int AddItem(const char *pName, const char *pSection, EXWATCH::DataType type, void *pAddr, unsigned int parent)
    {
        unsigned int section = SectionHandle(pSection, parent);
        return AddItem(pName, section, type, pAddr);
    }
    unsigned int AddSection(const char *pName, unsigned int parent, const char *pParent, unsigned int key)
    {
        if (!parent) parent = SectionHandle(pParent, parent);
        unsigned int h = AddSectionFunc(pName, parent);
        Section(h)->m_Key = key;
        return h;
    }
    unsigned int AddSection(const char *pName, const char *pParent, const char *pParent2, unsigned int key)
    {
        unsigned int parent = SectionHandle(pParent, 0);
        if (!parent) parent = SectionHandle(pParent2, parent);
        unsigned int h = AddSectionFunc(pName, parent);
        Section(h)->m_Key = key;
        return h;
    }
    void SetItemAddr(unsigned int h, void *pAddr);
    void DisableItemEdit(unsigned int h);
    void EnableItemEdit(unsigned int h, s8 save);
    void EnableItemEdit(unsigned int h, const ex::EXVar32 &min, const ex::EXVar32 &max, const ex::EXVar32 &step, s8 save);
    void EnableItemEdit(unsigned int h, unsigned int min, unsigned int max, unsigned int step, s8 save);
    void EnableItemEdit(unsigned int h, int min, int max, int step, s8 save);
    void EnableItemEdit(unsigned int h, float min, float max, float step, s8 save);
    EXWatchItem *WatchItem(unsigned int h) { return &m_WatchItems.ElementAt(h); }
protected:
    unsigned int AddSectionFunc(const char *pName, unsigned int parent);
    void RestoreItemValue(unsigned int h);

    static EXWatcher *m_pWatcher;
    EXDataArray<EXWatchItem, const EXWatchItem &> m_WatchItems;  // 0x2c
};
#endif
