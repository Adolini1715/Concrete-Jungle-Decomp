#include "EXDebugTree.h"
#include "EXString.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

extern "C" char *__cdecl strncpy(char *, const char *, unsigned int);

unsigned int EXDebugTree::SectionHandle(const char *pName, unsigned int parent)
{
    int n = m_Sections.Size();
    int i;
    for (i = 1; i < n; i++) {
        EXDebugTreeSection &s = m_Sections[i];
        if (s.m_pName == pName && !strcmp(pName, s.m_Name)) {
            if (!parent || parent == s.m_Key) return i;
        }
    }
    for (i = 1; i < n; i++) {
        EXDebugTreeSection &s = m_Sections[i];
        if (!strcmp(pName, s.m_Name)) {
            if (!parent || parent == s.m_Key) return i;
        }
    }
    return 0;
}

unsigned int EXDebugTree::AddSection(const char *pName, const char *pParent)
{
    return AddSection(pName, SectionHandle(pParent, 0), ADD_DUPLICATE_IGNORE);
}

unsigned int EXDebugTree::ItemHandle(const char *pName)
{
    int n = m_Items.Size();
    int i;
    for (i = 1; i < n; i++) {
        EXDebugTreeItem &it = m_Items[i];
        if (it.m_pName == pName && !strcmp(pName, it.m_Name)) return i;
    }
    for (i = 1; i < n; i++) {
        if (!strcmp(pName, m_Items[i].m_Name)) return i;
    }
    return 0;
}

// NEAR: 26 bytes differ, scheduling of the section address (retail keeps the section pointer).
unsigned int EXDebugTree::ItemHandle(const char *pName, unsigned int section)
{
    if (section && section < (unsigned int)m_Sections.Size()) {
        EXDebugTreeSection *pSection = &m_Sections[section];
        EXDListItem *pHead = pSection->m_Items.Head();
        EXDListItem *p;
        for (p = pHead; p; p = p->m_pNext) {
            EXDebugTreeItem *pItem = (EXDebugTreeItem *)p;
            if (pItem->m_pName == pName && !strcmp(pName, pItem->m_Name)) return pItem - m_Items.GetData();
        }
        for (p = pHead; p; p = p->m_pNext) {
            EXDebugTreeItem *pItem = (EXDebugTreeItem *)p;
            if (!strcmp(pName, pItem->m_Name)) return pItem - m_Items.GetData();
        }
        return 0;
    }
    return ItemHandle(pName);
}

unsigned int EXDebugTree::ItemHandle(const char *pName, const char *pSection)
{
    return ItemHandle(pName, SectionHandle(pSection, 0));
}

unsigned int EXDebugTree::AddItem(const char *pName, const char *pSection, unsigned int type)
{
    return AddItem(pName, SectionHandle(pSection, 0), type, ADD_DUPLICATE_IGNORE);
}

void EXDebugTree::DelItem(unsigned int h)
{
    if (h && h < (unsigned int)m_Items.Size()) DelItem(&m_Items[h]);
}

void EXDebugTree::DelSection(EXDebugTreeSection *pSection)
{
    EXDListItem *p, *pNext;
    for (p = pSection->m_Children.Head(); p; p = pNext) {
        pNext = p->m_pNext;
        DelSection((EXDebugTreeSection *)p);
    }
    for (p = pSection->m_Items.Head(); p; p = pNext) {
        pNext = p->m_pNext;
        DelItem((EXDebugTreeItem *)p);
    }
    m_Sections[pSection->m_Parent].m_Children.Remove(pSection);
    pSection->m_Parent = 0;
    pSection->m_pName = 0;
    pSection->m_Name[0] = 0;
    pSection->m_Flags = 2;
}

void EXDebugTree::DelItem(EXDebugTreeItem *pItem)
{
    m_Sections[pItem->m_Section].m_Items.Remove(pItem);
    pItem->m_Section = 0;
    pItem->m_Type = 0;
    pItem->m_pName = 0;
    pItem->m_Name[0] = 0;
    pItem->m_Flags = 1;
}

#define EX_REMAP(p, d) if (p) p = (EXDListItem *)((char *)(p) + (d))
// Both links are read before either is adjusted.
#define EX_REMAP2(a, b, d) \
    { \
        EXDListItem *pA = a, *pB = b; \
        if (pA) a = (EXDListItem *)((char *)pA + (d)); \
        if (pB) b = (EXDListItem *)((char *)pB + (d)); \
    }

void EXDebugTree::RemapSections(int delta)
{
    for (unsigned int i = 0; i < (unsigned int)m_Sections.Size(); i++) {
        EXDebugTreeSection &s = m_Sections[i];
        if (!(s.m_Flags & 2)) {
            EX_REMAP2(s.m_Children.m_pHead, s.m_Children.m_pTail, delta);
            EX_REMAP2(s.m_pPrev, s.m_pNext, delta);
        }
    }
}

void EXDebugTree::RemapItems(int delta)
{
    unsigned int i;
    for (i = 0; i < (unsigned int)m_Sections.Size(); i++) {
        EXDebugTreeSection &s = m_Sections[i];
        if (!(s.m_Flags & 2)) {
            EX_REMAP2(s.m_Items.m_pHead, s.m_Items.m_pTail, delta);
        }
    }
    for (i = 1; i < (unsigned int)m_Items.Size(); i++) {
        EXDebugTreeItem &it = m_Items[i];
        if (!(it.m_Flags & 1)) {
            EX_REMAP2(it.m_pPrev, it.m_pNext, delta);
        }
    }
}

unsigned int EXInfoTree::AddInfoItem(const char *pName, unsigned int section, unsigned int value, s8 flag, ADD_DUPLICATE_ACTION action)
{
    unsigned int h = AddItem(pName, section, (flag ? 0x80000000 : 0) | 0, action);
    if (h > 0) m_Items[h].m_Value = value;
    return h;
}

unsigned int EXInfoTree::AddInfoItem(const char *pName, const char *pSection, unsigned int value, s8 flag, ADD_DUPLICATE_ACTION action)
{
    return AddInfoItem(pName, SectionHandle(pSection, 0), value, flag, action);
}

unsigned int EXInfoTree::AddInfoItem(const char *pName, unsigned int section, float value, s8 flag, ADD_DUPLICATE_ACTION action)
{
    unsigned int h = AddItem(pName, section, (flag ? 0x80000000 : 0) | 1, action);
    if (h > 0) m_Items[h].m_Value = *(u32 *)&value;
    return h;
}

unsigned int EXInfoTree::AddInfoItem(const char *pName, const char *pSection, float value, s8 flag, ADD_DUPLICATE_ACTION action)
{
    return AddInfoItem(pName, SectionHandle(pSection, 0), value, flag, action);
}

unsigned int EXInfoTree::AddInfoItem(const char *pName, unsigned int section, const char *pValue, s8 flag, ADD_DUPLICATE_ACTION action)
{
    unsigned int h = AddItem(pName, section, (flag ? 0x80000000 : 0) | 2, action);
    if (h > 0) {
        EXDebugTreeItem *pItem = &m_Items[h];
        EXString *pStr = (EXString *)pItem->m_Value;
        if (!pStr) {
            pStr = new EXString;
            pItem->m_Value = (u32)pStr;
        }
        *pStr = pValue;
    }
    return h;
}

unsigned int EXInfoTree::AddInfoItem(const char *pName, const char *pSection, const char *pValue, s8 flag, ADD_DUPLICATE_ACTION action)
{
    return AddInfoItem(pName, SectionHandle(pSection, 0), pValue, flag, action);
}

void EXInfoTree::Empty()
{
    for (unsigned int i = 1; i < (unsigned int)m_Sections.Size(); i++) {
        if (!(m_Sections[i].m_Flags & 2)) DelSection(i);
    }
}

EXDebugTree::~EXDebugTree() {}

void EXDebugTree::DelSection(unsigned int h)
{
    if (h && h < (unsigned int)m_Sections.Size()) DelSection(&m_Sections[h]);
}

// NEAR: register swap of the size/pointer pair in the free-slot loop, store order of the new item.
unsigned int EXDebugTree::AddItem(const char *pName, unsigned int section, unsigned int type, ADD_DUPLICATE_ACTION action)
{
    unsigned int h = ItemHandle(pName, section);
    s8 update = 0;
    s8 add = 1;
    switch (action) {
    case ADD_DUPLICATE_ALWAYS:
        break;
    case ADD_DUPLICATE_UPDATE:
        if (h) update = 1;
        break;
    default:
        if (h) add = 0;
        break;
    }
    if (!section || section >= (unsigned int)m_Sections.Size()) return 0;
    if (update) {
        m_Items[h].m_Type = type;
        return h;
    }
    if (add) {
        int n = m_Items.Size();
        h = 0;
        for (int i = 1; i < n; i++) {
            if (m_Items[i].m_Flags & 1) {
                h = i;
                break;
            }
        }
        if (!h) {
            EXDebugTreeItem *pOld = m_Items.GetData();
            h = n;
            m_Items.SetSize(h + 1, -1);
            if (pOld != m_Items.GetData()) RemapItems((char *)m_Items.GetData() - (char *)pOld);
        }
        EXDebugTreeItem *pItem = &m_Items[h];
        pItem->m_pName = pName;
        strncpy(pItem->m_Name, pName, 14);
        pItem->m_Name[14] = 0;
        pItem->m_Flags = 0;
        pItem->m_Type = type;
        pItem->m_Value = 0;
        pItem->m_Section = section;
        Section(section)->m_Items.AddTail(pItem);
    }
    return h;
}

void EXInfoTree::DelItem(EXDebugTreeItem *pItem)
{
    if ((pItem->m_Type & 0x7fffffff) == 2) {
        EXString *pStr = (EXString *)pItem->m_Value;
        if (pStr) delete pStr;
        pItem->m_Value = 0;
    }
    EXDebugTree::DelItem(pItem);
}

EXDebugTree::EXDebugTree(unsigned int nSections, unsigned int nItems)
{
    m_Sections.SetSize(nSections, -1);
    m_Sections.SetSize(1, nSections);
    m_Items.SetSize(nItems, -1);
    m_Items.SetSize(1, nItems);
    EXDebugTreeSection &root = m_Sections[0];
    root.m_pName = "*ROOT*";
    strncpy(root.m_Name, "*ROOT*", 14);
    root.m_Name[14] = 0;
    root.m_Flags = 0;
}

// NEAR: same register swap as AddItem, parent list address folded.
unsigned int EXDebugTree::AddSection(const char *pName, unsigned int parent, ADD_DUPLICATE_ACTION action)
{
    unsigned int h = SectionHandle(pName, 0);
    switch (action) {
    case ADD_DUPLICATE_ALWAYS:
        break;
    case ADD_DUPLICATE_UPDATE:
        if (h) {
            EXDebugTreeSection *pSection = &m_Sections[h];
            pSection->m_pName = pName;
            strncpy(pSection->m_Name, pName, 14);
            pSection->m_Name[14] = 0;
            return h;
        }
        break;
    default:
        if (h) return h;
        break;
    }
    int n = m_Sections.Size();
    h = 0;
    for (int i = 1; i < n; i++) {
        if (m_Sections[i].m_Flags & 2) {
            h = i;
            break;
        }
    }
    if (!h) {
        // Retail checks the item array here (copy of AddItem).
        EXDebugTreeItem *pOld = m_Items.GetData();
        h = n;
        m_Sections.SetSize(h + 1, -1);
        if (pOld != m_Items.GetData()) RemapItems((char *)m_Items.GetData() - (char *)pOld);
    }
    EXDebugTreeSection *pSection = &m_Sections[h];
    pSection->m_pName = pName;
    strncpy(pSection->m_Name, pName, 14);
    pSection->m_Name[14] = 0;
    pSection->m_Flags = 0;
    pSection->m_Parent = parent;
    Section(parent)->m_Children.AddTail(pSection);
    return h;
}

EXInfoTree::EXInfoTree(unsigned int nSections, unsigned int nItems) : EXDebugTree(nSections, nItems) {}
