#ifndef EXDEBUGTREE_H
#define EXDEBUGTREE_H
#include "types.h"
#include "EXList.h"
#include "EXArray.h"
#include "EXNew.h"

// Tree node for a section (sizeof 0x30). Index 0 is the root ("*ROOT*").
struct EXDebugTreeSection : public EXDListItem {
    EXDebugTreeSection() {}
    u16 m_Parent;                     // 0x08 parent section index
    u16 m_Key;                        // 0x0a group key (EXWatcher::AddSection); SectionHandle filters on it
    EXDList m_Children;               // 0x0c child sections
    EXDList m_Items;                  // 0x14 items
    const char *m_pName;              // 0x1c the caller's string (fast compare)
    EXFixedArray<char, 15> m_Name;    // 0x20 copy, 14 chars + 0
    union {
        u8 m_Flags;                   // 0x2f
        struct {
            u8 m_Open : 1;            // expanded in the watcher window
            u8 m_Free : 1;
        };
    };
};

// Leaf (sizeof 0x28).
struct EXDebugTreeItem : public EXDListItem {
    u16 m_Section;                    // 0x08 owning section index
    u16 m_Unk0a;
    u32 m_Type;                       // 0x0c data type (bit 31: info flag)
    u32 m_Value;                      // 0x10 value / address / EXString *
    const char *m_pName;              // 0x14
    EXFixedArray<char, 15> m_Name;    // 0x18
    union {
        u8 m_Flags;                   // 0x27
        struct {
            u8 m_Free : 1;
            u8 m_Unk1 : 3;
            u8 m_Edit : 4;            // 1 editable, 2 value saved
        };
    };
};

// Named sections and items in fixed-stride arrays; handles are array indices (0 = none).
class EXDebugTree {
public:
    enum ADD_DUPLICATE_ACTION { ADD_DUPLICATE_IGNORE = 0, ADD_DUPLICATE_ALWAYS = 1, ADD_DUPLICATE_UPDATE = 2 };
    virtual ~EXDebugTree();
    // Overloaded virtuals: VC places them in reverse declaration order.
    virtual unsigned int AddSection(const char *pName, unsigned int parent, ADD_DUPLICATE_ACTION action);
    virtual unsigned int AddSection(const char *pName, const char *pParent);
    virtual void DelSection(unsigned int h);
    virtual unsigned int AddItem(const char *pName, unsigned int section, unsigned int type, ADD_DUPLICATE_ACTION action);
    virtual unsigned int AddItem(const char *pName, const char *pSection, unsigned int type);
    virtual void DelItem(unsigned int h);
protected:
    virtual void DelItem(EXDebugTreeItem *pItem);
public:
    EXDebugTree(unsigned int nSections, unsigned int nItems);
    EXDebugTreeSection *Section(unsigned int h) { return &m_Sections[h]; }
    EXDebugTreeItem *Item(unsigned int h) { return &m_Items[h]; }
    int NumSections() const { return m_Sections.Size(); }
    unsigned int SectionHandle(const char *pName, unsigned int parent);
    unsigned int ItemHandle(const char *pName);
    unsigned int ItemHandle(const char *pName, unsigned int section);
    unsigned int ItemHandle(const char *pName, const char *pSection);
protected:
    void DelSection(EXDebugTreeSection *pSection);
    void RemapSections(int delta);
    void RemapItems(int delta);

    EXArray<EXDebugTreeSection, const EXDebugTreeSection &> m_Sections;  // 0x04
    EXArray<EXDebugTreeItem, const EXDebugTreeItem &> m_Items;           // 0x18
};

// Debug tree whose items carry values (u32, float or an owned EXString).
class EXInfoTree : public EXDebugTree {
public:
    EXInfoTree(unsigned int nSections, unsigned int nItems);
    unsigned int AddInfoItem(const char *pName, unsigned int section, unsigned int value, s8 flag, ADD_DUPLICATE_ACTION action);
    unsigned int AddInfoItem(const char *pName, const char *pSection, unsigned int value, s8 flag, ADD_DUPLICATE_ACTION action);
    unsigned int AddInfoItem(const char *pName, unsigned int section, float value, s8 flag, ADD_DUPLICATE_ACTION action);
    unsigned int AddInfoItem(const char *pName, const char *pSection, float value, s8 flag, ADD_DUPLICATE_ACTION action);
    unsigned int AddInfoItem(const char *pName, unsigned int section, const char *pValue, s8 flag, ADD_DUPLICATE_ACTION action);
    unsigned int AddInfoItem(const char *pName, const char *pSection, const char *pValue, s8 flag, ADD_DUPLICATE_ACTION action);
    void Empty();
protected:
    virtual void DelItem(EXDebugTreeItem *pItem);
};
#endif
